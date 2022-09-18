#ifdef lint
static char *sccsid = "@(#)tstp.c	4.1	(ULTRIX)	7/2/90";
#endif lint

# include	<signal.h>

# ifdef SIGTSTP

# include	"curses.ext"

/*
 * handle stop and start signals
 *
 * 3/5/81 (Berkeley) @(#)_tstp.c	1.1
 */
_tstp() {
	int omask; 

# ifdef DEBUG
	if (outf) fflush(outf);
# endif

#define mask(s) (1 << ((s)-1)) 
	
	/* _ll_move(lines-1, 0); */
	endwin();
	fflush(stdout);

	signal(SIGTSTP, SIG_DFL);
 /* AKR - changes for V2.2 */
	   	/* reset signal handler so */
		/* kill stops below us */
	omask = sigsetmask(sigblock(0) &~ mask(SIGTSTP));
	kill(0, SIGTSTP);
	sigblock(mask(SIGTSTP));  
	signal(SIGTSTP, _tstp);
	fixterm();
	SP->doclear = 1;
	refresh();
}
# endif
