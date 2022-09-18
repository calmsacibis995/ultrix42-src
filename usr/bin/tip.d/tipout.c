#ifndef lint
static char sccsid[] = "@(#)tipout.c	4.1	(ULTRIX)	7/17/90";
#endif

#include "tip.h"
/*
 * tip
 *
 * lower fork of tip -- handles passive side
 *  reading from the remote host
 *
 *	EDIT HISTORY
 *	9-Mar-1986  Marc Teitelbaum -0001
 *	   Change routine called via SIGHUP to be finish() instead
 *	   of intTERM().
 */

jmp_buf sigbuf;

/*
 * TIPOUT wait state routine --
 *   sent by TIPIN when it wants to posses the remote host
 */
intIOT()
{
	longjmp(sigbuf, 1);
}

/*
 * Scripting command interpreter --
 *  accepts script file name over the pipe and acts accordingly
 */
char scriptfl[256];
intEMT()
{
	if (boolean(value(SCRIPT)) && fscript != NULL)
		fclose(fscript);
	if ((fscript = fopen(value(RECORD), "a")) == NULL)
		printf("can't create %s\r\n", value(RECORD));
	else
		boolean(value(SCRIPT)) = TRUE;
	longjmp(sigbuf, 1);
}

intTERM()
{

	if (boolean(value(SCRIPT)) && fscript != NULL)
		fclose(fscript);
	exit(0);
}

intSYS()
{

	boolean(value(BEAUTIFY)) = !boolean(value(BEAUTIFY));
	longjmp(sigbuf, 1);
}

/*
 * ****TIPOUT	TIPOUT****
 */
tipoutinit() {
	extern finish();

	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGEMT, intEMT); 	/* attention from TIPIN */
	signal(SIGTERM, intTERM);	/* time to go signal */
	signal(SIGIOT, intIOT); 	/* scripting going on signal */
	signal(SIGHUP, finish);		/* for dial-ups, marc-0001 */
	signal(SIGSYS, intSYS); 	/* beautify toggle */

}

#define mask(s) (1 << ((s) - 1))
#define ALLSIGS mask(SIGEMT)|mask(SIGTERM)|mask(SIGIOT)|mask(SIGSYS)
#define DZBUF 64
tipout()
{
	unsigned char buf[DZBUF];
	register unsigned char *cp;
	register int cnt;
	register int omask;
	caddr_t incnt;
	int tot;



	cnt = read(FD, buf, DZBUF);
	(void) write(1, buf, cnt);
	omask = sigblock(ALLSIGS);
	if (boolean(value(SCRIPT)) && fscript != NULL) {
	    if (!boolean(value(EIGHTBIT)))
		for (cp = buf; cp < buf + cnt; cp++)
			*cp &= 0177;
	    if (!boolean(value(BEAUTIFY))) {
		fwrite(buf, 1, cnt, fscript);
		sigsetmask(omask);
		return;
	    }
	    for (cp = buf; cp < buf + cnt; cp++)
		if ((*cp >= ' ' && *cp <= '~') ||
		    (boolean(value(EIGHTBIT)) && (*cp >= 0xa0 && *cp <= 0xff)) ||
		    any(*cp, value(EXCEPTIONS)))
		    putc(*cp, fscript);
	}
	/* This trys to get us a second buffer if its there */
	(void) ioctl(FD, FIONREAD, (caddr_t) &incnt);
	if(incnt > 0) {
	    cnt = read(FD, buf, ((int) incnt <= DZBUF ? (int) incnt : DZBUF));
	    (void) write(1, buf, cnt);
	    if (boolean(value(SCRIPT)) && fscript != NULL) {
		for (cp = buf; cp < buf + cnt; cp++)
		    *cp &= 0177;
		if (!boolean(value(BEAUTIFY))) {
		    fwrite(buf, 1, cnt, fscript);
		    sigsetmask(omask);
		    return;
		}
		for (cp = buf; cp < buf + cnt; cp++)
		    if ((*cp >= ' ' && *cp <= '~') ||
			(boolean(value(EIGHTBIT)) && (*cp >= 0xa0 && *cp <= 0xff)) ||
			any(*cp, value(EXCEPTIONS)))
			putc(*cp, fscript);
	    }
	}
	sigsetmask(omask);
}
