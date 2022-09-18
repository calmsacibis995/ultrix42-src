/*  Modification Record 
 *
 *  7/30/86	Aya Konishi (konishi@gully)
 *	ICA-01035
 *	Changed handling of intrpt() so that control-D at the
 * 	shell prompt would not get into an infinite loop.
 *
 */
#ifndef lint
static char sccsid[] = "@(#)learn.c	4.1	ULTRIX	7/17/90";
/*static char sccsid[] = "@(#)learn.c	4.2	(Berkeley)	4/27/83";*/
#endif not lint

#include "stdio.h"
#include "lrnref.h"
#include "signal.h"

char	*direct	= "/usr/lib/learn";	/* CHANGE THIS ON YOUR SYSTEM */
int	more;
char	*level;
int	speed;
char	*sname;
char	*todo;
FILE	*incopy	= NULL;
int	didok;
int	sequence	= 1;
int	comfile	= -1;
int	status;
int	wrong;
char	*pwline;
char	*dir;
FILE	*scrin;
int	logging	= 1;	/* set to 0 to turn off logging */
int	ask;
int	again;
int	skip;
int	teed;
int	total;

main(argc,argv)
int argc;
char *argv[];
{
	extern hangup(), intrpt();
	extern char * getlogin();
	char *malloc();

	speed = 0;
	more = 1;
	pwline = getlogin();
	setlinebuf(stderr);
	selsub(argc, argv);
	chgenv();
	signal(SIGHUP, hangup);
	signal(SIGINT, intrpt);
	while (more) {
		selunit();
		dounit();
		whatnow();
	}
	wrapup(0);
}

hangup()
{
	wrapup(1);
}

intrpt()
{
	char response[20];
	char ans;

	signal(SIGINT, hangup);
ONCEMORE:	fprintf(stderr, "\nInterrupt.\nWant to go on?  ");
	if (gets(response) == NULL) 
		wrapup(0);/*  Bug fix by Aya. ICA-01035 */
	else if (((ans = response[0]) == 'n') | (ans == 'N'))
		wrapup(0);
	else if ((ans == 'y') | (ans == 'Y')) 
		return(0);
	else {
		fprintf(stderr,"Please respond with 'y' or 'n'.\n");
		goto ONCEMORE;
	}
	signal(SIGINT, intrpt);
}
