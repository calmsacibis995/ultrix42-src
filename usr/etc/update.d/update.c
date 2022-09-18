#ifndef lint
static	char	*sccsid = "@(#)update.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/*
 * Update the file system every n seconds (command line arg.).
 * For cache benefit, open certain system directories.
 */

/* 
 * Modification History:
 *
 * 22 Sep 88 -- chet
 *	Make the previous change work correctly.
 *
 * 13-Jun-88 -- chet
 *	Added a command line argument which allows you to specify the
 *	number of seconds between updates. The default is 30 seconds.
 *	The acceptable range is given by the define constants MINSYNC
 *	& MAXSYNC.
 *
 * 09-Jun-88	Mark Parenti
 *	Changed signal handlers to void.
 *
 */

#include <signal.h>

#define DFLTSYNC 30	/* default time between syncs (in seconds) */
#define MINSYNC 10	/* min time allowed between syncs (in seconds) */
#define MAXSYNC 600	/* max time allowed between syncs (in seconds) */

void dosync();
char *fillst[] = {
	"/bin",
	"/lib",
	"/usr",
	"/var",
	"/usr/bin",
	"/usr/lib",
	"/usr/ucb",
	"/var/spool",
	0,
};

int	interval;	/* time interval, in seconds, between updates */

main(argc, argv)
int	argc;
char	**argv;
{
	char	**f;

	/*
	 * If sync interval given, then use it.
	 * Assure that its bounds are reasonable.
	 */
	if (argc > 1) {
		interval = atoi(argv[1]);
		if (interval < MINSYNC)
			interval = MINSYNC;
		else if (interval > MAXSYNC)
			interval = MAXSYNC;
	} else {
		interval = DFLTSYNC;
	};

	if(fork())
		exit(0);
	close(0);
	close(1);
	close(2);
	for(f = fillst; *f; f++)
		open(*f, 0);
	dosync();
	for(;;)
		pause();
}

void
dosync()
{
	sync();
	signal(SIGALRM, dosync);
	alarm(interval);
}
