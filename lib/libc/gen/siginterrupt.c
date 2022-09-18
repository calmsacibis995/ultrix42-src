#ifndef lint
static char *sccsid = "@(#)siginterrupt.c	4.1	ULTRIX	7/3/90";
#endif not lint

#include <signal.h>

/*
 * Set signal state to prevent restart of system calls
 * after an instance of the indicated signal.
 */
siginterrupt(sig, flag)
	int sig, flag;
{
	struct sigvec sv;
	register int ret;

	if ((ret = sigvec(sig, (struct sigvec *)0, &sv)) < 0)
		return (ret);
	if (flag)
		sv.sv_flags |= SV_INTERRUPT;
	else
		sv.sv_flags &= ~SV_INTERRUPT;
	return (sigvec(sig, &sv, (struct sigvec *)0));
}
