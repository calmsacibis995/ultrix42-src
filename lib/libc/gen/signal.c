#ifndef lint
static char *sccsid = "@(#)signal.c	4.1	ULTRIX	7/3/90";
#endif not lint
/*
 * Almost backwards compatible signal.
 */
/*
 *	Modification History
 *
 * 13-Jan-1988 -- map
 *	Set SV_OLDSIG flag so kernel can perform System V/POSIX behavior.
 *	In BSD mode this flag will have no effect.
 */

#include <signal.h>
#include <errno.h>


void (*
signal(s, a))()
	register int s;
	void  (*a)();
{
	struct sigvec osv, sv;
	static int mask[NSIG];		/* saved across calls */
	static int flags[NSIG];		/* saved across calls */
	extern int errno;

	if (s < 0 || s >= NSIG) {
		errno = EINVAL;
		return (BADSIG);
	}
	sv.sv_handler = a;
	sv.sv_mask = mask[s];
	sv.sv_flags = flags[s];
	sv.sv_flags |= SV_OLDSIG; 	/* Mark as called from signal() */
	if (sigvec(s, &sv, &osv))	/* try it in one syscall */
		return (BADSIG);
	if (sv.sv_mask != osv.sv_mask || sv.sv_flags != osv.sv_flags) {
	/* something is different or 1st time thru - save off mask and flags */
		mask[s]     = osv.sv_mask;
		sv.sv_mask  = osv.sv_mask;
		flags[s]    = osv.sv_flags;
		sv.sv_flags = osv.sv_flags;
		sv.sv_flags |= SV_OLDSIG; 	/* Mark as called from signal() */
			if (sigvec(s, &sv, (struct sigvec *)0) < 0)
			return (BADSIG);
	}
	return (osv.sv_handler);
}
