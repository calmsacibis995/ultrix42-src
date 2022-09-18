

#ifndef lint
static	char	*sccsid = "@(#)setsig.c	4.1	(ULTRIX)	7/17/90";
#endif lint

# include	"signal.h"
# include	"sys/types.h"
# include	"macros.h"

#define ONSIG	16

/*
	General-purpose signal setting routine.
	All non-ignored, non-caught signals are caught.
	If a signal other than hangup, interrupt, or quit is caught,
	a "user-oriented" message is printed on file descriptor 2 with
	a number for help(I).
	If hangup, interrupt or quit is caught, that signal	
	is set to ignore.
	Termination is like that of "fatal",
	via "clean_up(sig)" (sig is the signal number)
	and "exit(userexit(1))".
 
	If the file "dump.core" exists in the current directory
	the function commits
	suicide to produce a core dump
	(after calling clean_up, but before calling userexit).
*/


char	*Mesg[ONSIG]={
	0,
	0,	/* Hangup */
	0,	/* Interrupt */
	0,	/* Quit */
	"Illegal instruction",
	"Trace/BPT trap",
	"IOT trap",
	"EMT trap",
	"Floating exception",
	"Killed",
	"Bus error",
	"Memory fault",
	"Bad system call",
	"Broken pipe",
	"Alarm clock"
};


setsig()
{
	extern int setsig1();
	register int j;
	register void (*n)();

	for (j=1; j<ONSIG; j++)
		if ((n=signal(j,setsig1))!=SIG_DFL)	/* DAG */
			signal(j,n);
}


static char preface[]="SIGNAL: ";
static char endmsg[]=" (ut12)\n";

setsig1(sig)
int sig;
{

	if (Mesg[sig]) {
		write(2,preface,length(preface));
		write(2,Mesg[sig],length(Mesg[sig]));
		write(2,endmsg,length(endmsg));
	}
	else
		signal(sig,SIG_IGN);
	clean_up(sig);
	if(open("dump.core",0) > 0) {
		signal(SIGIOT,SIG_DFL);
		abort();
	}
	exit(userexit(1));
}
