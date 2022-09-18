#ifndef lint
static	char	*sccsid = "@(#)sleep.c	4.1	(ULTRIX)	7/3/90";
#endif

/*	Modification History						
 *									
 *	001 - Mark A. Parenti						
 *		Rewrote to return unslept time.  Also fixed problem	
 *		where alarm would not be sent until AFTER the sleep	
 *		had returned.						
 *
 *	002 - Mark A. Parenti
 *		Use getsysinfo() to determine environment.  POSIX and SYSV
 *		sleep() is interrupted by any signal.  BSD sleep() is not
 *		interrupted by signals.
 *
 *	003 - Mark A. Parenti
 *		Delete mask macro and use the equivalent sigmask macro
 *		instead.
 *
 *	004 - Mark A. Parenti
 *		Fix bug which caused sleep() to hang if SIGALRM was ignored
 *		and a previous alarm would shorten sleep().  The fix is to
 *		use kill() instead of setitimer() to send SIGALRM to process.
 *
 *	004 - Mark A. Parenti
 *		Change progenv to short because getsysinfo() only copyout's
 *		a short from the kernel.
 *
 *	005 - dws
 *		Fixed handling of unslept amount.
 */
#include <sys/time.h>
#include <sys/exec.h>
#include <sys/sysinfo.h>
#include <signal.h>

#define	setvec(vec, a) \
	vec.sv_handler = a; vec.sv_mask = vec.sv_onstack = 0

static int ringring;

unsigned
sleep(n)
	unsigned n;
{
	void sleepx();
	int omask;
	int alrm_flg = 0;
	struct itimerval itv, oitv, ritv;
	register struct itimerval *itp = &itv;
	struct sigvec vec, ovec;
	unsigned unslept, left_ovr;
	short	progenv;

	if (n == 0)
		return;
	timerclear(&itp->it_interval);
	timerclear(&itp->it_value);
	if (setitimer(ITIMER_REAL, itp, &oitv) < 0)
		return;

	left_ovr = 0;
	unslept = 0;
	if( getsysinfo(GSI_PROG_ENV, &progenv, sizeof(int), 0, 0, 0) < 1 )
		progenv = A_BSD;
	setvec(ovec, SIG_DFL);
	omask = sigblock(0);
	itp->it_value.tv_sec = n;
	if (timerisset(&oitv.it_value)) {
		if (timercmp(&oitv.it_value, &itp->it_value, >)) {
			alrm_flg = 1;
			oitv.it_value.tv_sec -= itp->it_value.tv_sec;
		}
		else {
			alrm_flg = 2;
			left_ovr =  itp->it_value.tv_sec - oitv.it_value.tv_sec;
			itp->it_value = oitv.it_value;
			oitv.it_value.tv_sec = 0;
			oitv.it_value.tv_usec = 200;
		}
	}
	setvec(vec, sleepx);
	(void) sigvec(SIGALRM, &vec, &ovec);
	ringring = 0;
	sigsetmask(-1); /* Block all signals while we setup alarm */
	(void) setitimer(ITIMER_REAL, itp, (struct timeval *)0);
	/* In BSD mode signals do not interrupt sleep.  In POSIX and System V
	 * mode, any caught signal interrupts the sleep.
	 */
	if(progenv == A_BSD) {
	    while (!ringring)
		sigpause(omask &~ sigmask(SIGALRM));
	}
	else
	    sigpause(omask);
	sigsetmask(omask); /* Reset signal mask */
	(void) sigvec(SIGALRM, &ovec, (struct sigvec *)0);
	(void) getitimer(ITIMER_REAL, &ritv);
	unslept = ritv.it_value.tv_sec;	/* Unslept amount, if any */
	/*
	 * If alarm shortened sleep, then send SIGALRM before 
	 * we return. If alarm is past sleep then restart
	 * alarm with remaining time.  If no previous alarm
	 * but unslept time then reset timer.
	 */
	switch (alrm_flg) {
	case 2  : /* previous timer shorter than sleep time */
		kill(getpid(), SIGALRM);
		break;
	case 1  : /* previous timer longer than sleep time */
		oitv.it_value.tv_sec += unslept;
		(void) setitimer(ITIMER_REAL, &oitv, (struct timeval *)0);
		break;
	case 0  : /* no previous timer */
	default :
		if (unslept > 0) { /* Received unexpected SIGALRM */
			timerclear(&oitv.it_value);
			(void) setitimer(ITIMER_REAL, &oitv, (struct timeval *)0);
		}
		break;
	}

	/*
	 *  Mark Parenti
	 *  Change to return unslept amount (if any).
	 *  POSIX requirement.
	 */
	return(left_ovr + unslept);
}

static void
sleepx()
{

	ringring = 1;
}

