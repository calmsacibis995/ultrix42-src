#ifndef lint
static	char	*sccsid = "@(#)runpcs.c	4.1	(ULTRIX)	11/23/87";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/************************************************************************
 *
 *			Modification History
 *
 *	David L Ballenger, 16-May-1985
 * 003	Fix for QPR-00140
 *	Fix handling of arguments on the :r command line, so that quoted
 *	arguments are handled correctly.   Also, allow arguments / file
 *	names to be directly adjacent to i/o redirection tokens with no
 *	intervening white space.  Improve error messages for i/o
 *	redirection errors.
 *									
 *	David L Ballenger, 15-Apr-1985					
 * 002	Remove definition of errno, since it is now defined in 		
 *	<sys/errno.h>							
 *									
 *	Stephen Reilly, 18-Feb-84
 * 001- In the case of ":r arg1 arg2 arg3 ..." the first character of
 *	the argumented starting at arg2 and on where being dropped.
 *
 ***********************************************************************/

/*
 *
 *	UNIX debugger
 *
 */

#include "defs.h"
#include <stdio.h>
#include <sys/file.h>

extern	MAP	txtmap;

MSG		NOFORK;
MSG		ENDPCS;
MSG		BADWAIT;

CHAR		*lp;
ADDR		sigint;
ADDR		sigqit;

/* breakpoints */
BKPTR		bkpthead;

REGLIST		reglist[];

CHAR		lastc;

INT		fcor;
INT		fsym;
STRING		errflg;
INT		signo;
INT		sigcode;

L_INT		dot;
STRING		symfil;
INT		wtflag;
L_INT		pid;
L_INT		expv;
INT		adrflg;
L_INT		loopcnt;





/* service routines for sub process control */

getsig(sig)
{	return(expr(0) ? expv : sig);
}

ADDR userpc = 1;

runpcs(runmode,execsig)
{
	INT		rc;
	REG BKPTR	bkpt;
	IF adrflg THEN userpc=dot; FI
	printf("%s: running\n", symfil);

	WHILE --loopcnt>=0
	DO
#ifdef DEBUG
		printf("\ncontinue %x %d\n",userpc,execsig);
#endif
		IF runmode==SINGLE
		THEN delbp(); /* hardware handles single-stepping */
		ELSE /* continuing from a breakpoint is hard */
			IF bkpt=scanbkpt(userpc)
			THEN execbkpt(bkpt,execsig); execsig=0;
			FI
			setbp();
		FI
		ptrace(runmode,pid,userpc,execsig);
		bpwait(); chkerr(); execsig=0; delbp(); readregs();

		IF (signo==0) ANDF (bkpt=scanbkpt(userpc))
		THEN /* stopped by BPT instruction */
#ifdef DEBUG
			printf("\n BPT code; '%s'%o'%o'%d",
				bkpt->comm,bkpt->comm[0],EOR,bkpt->flag);
#endif
			dot=bkpt->loc;
			IF bkpt->flag==BKPTEXEC
			ORF ((bkpt->flag=BKPTEXEC)
				ANDF bkpt->comm[0]!=EOR
				ANDF command(bkpt->comm,':')
				ANDF --bkpt->count)
			THEN execbkpt(bkpt,execsig); execsig=0; loopcnt++;
			ELSE bkpt->count=bkpt->initcnt; rc=1;
			FI
		ELSE execsig=signo; rc=0;
		FI
	OD
	return(rc);
}

#define BPOUT 0
#define BPIN 1
INT bpstate = BPOUT;

endpcs()
{
	REG BKPTR	bkptr;
	IF pid
	THEN ptrace(EXIT,pid,0,0); pid=0; userpc=1;
	     FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
	     DO IF bkptr->flag
		THEN bkptr->flag=BKPTSET;
		FI
	     OD
	FI
	bpstate=BPOUT;
}

#ifdef VFORK
nullsig()
{

}
#endif

setup()
{
	close(fsym); fsym = -1;
#ifndef VFORK
	IF (pid = fork()) == 0
#else
	IF (pid = vfork()) == 0
#endif
	THEN ptrace(SETTRC,0,0,0);
#ifdef VFORK
	     signal(SIGTRAP,nullsig);
#endif
	     signal(SIGINT,sigint); signal(SIGQUIT,sigqit);
	     doexec(); exit(0);
	ELIF pid == -1
	THEN error(NOFORK);
	ELSE bpwait(); readregs(); lp[0]=EOR; lp[1]=0;
	     fsym=open(symfil,wtflag);
	     IF errflg
	     THEN printf("%s: cannot execute\n",symfil);
		  endpcs(); error(0);
	     FI
	FI
	bpstate=BPOUT;
}

execbkpt(bkptr,execsig)
BKPTR	bkptr;
{
#ifdef DEBUG
	printf("exbkpt: %d\n",bkptr->count);
#endif
	delbp();
	ptrace(SINGLE,pid,bkptr->loc,execsig);
	bkptr->flag=BKPTSET;
	bpwait(); chkerr(); readregs();
}

/* getarg
 *
 *	Get the next argument for the :r command line.  Handle simple quoted
 *	strings.
 */
static STRING
getarg(argptr)
	REG STRING argptr;
{
	/* Scan for end of argument.
	 */
	WHILE lastc != EOR ANDF lastc != SP ANDF lastc != TB ANDF
	      lastc != '>' ANDF lastc != '<'
	DO
		/* Check for quoted string
		 */
		IF lastc == SINGLE_QUOTE ORF lastc == DOUBLE_QUOTE THEN
			char quote = lastc;
			readchar();
			WHILE lastc != quote DO
				IF lastc == EOR THEN
					fprintf(stderr,
						"adb: missing %c.\n",quote);
					_exit(0);
				FI
				*argptr++ = lastc;
				readchar();
			OD
		ELSE
			/* Append unquoted characters to argument.
			 */
			*argptr++ = lastc;
		FI
		readchar();
	OD
	*argptr++ = '\0';
	return(argptr);
}

/* redirect_io:
 *
 *	Redirect standard input/output from/to a file.
 */
static void
redirect_io(fd)
	int fd;	/* file descriptor to be redirected */
{
	TYPE struct {
		int	flags,		/* Flags for open() */
			mode;		/* mode for open() */
		STRING	direction,	/* text for error messages */
			action;		/* text for error messages */
	} REDIRECTION_INFO;

	static REDIRECTION_INFO static_info[] = {
		{ O_RDONLY,   0, "input",  "open" },
		{ O_CREAT|O_TRUNC,  666, "output", "create"}
	};

	REG REDIRECTION_INFO *io = &static_info[fd];
	CHAR filename[LINSIZ];

	/* Skip redirection token and any subsequent whitespace,
	 * then get the filename.
	 */
	rdc();
	getarg(filename);

	/* Complain if there is no file name.
	 */
	IF strlen(filename) == 0 THEN
		fprintf(stderr,
			"adb: no file given for %s redirection\n",
			io->direction);
		_exit(0);
	FI

	/* Attempt to redirect the io to/from the specified file
	 */
	close(fd);
	IF open(filename,io->flags,io->mode)<0 THEN
		fprintf(stderr,
			"adb: cannot %s file %s for %s redirection\n",
			io->action,filename,io->direction);
		perror("adb");
		_exit(0);
	FI

}

/* doexec:
 *
 *	Get the arguments from the :r command line and exec the image
 *	being debugged.
 */
doexec()
{
	STRING		argl[MAXARG];
	CHAR		args[LINSIZ];
	REG STRING	*ap;
	extern   STRING *environ;

	ap = argl;	/* Point to argument list */
	*ap++ = symfil;	/* First arg is the symbol file */
	*ap = args;	/* Subsequent args are in "args" */
	readchar();	/* Skip 'r' command */

	WHILE lastc != EOR DO
		/*
		 * If we hit < or > then redirect the i/o accordingly
		 * using the file name which follows.
		 */
		IF   lastc == '<' THEN redirect_io(0);
		ELIF lastc == '>' THEN redirect_io(1);
		ELIF lastc == SP ORF lastc == TB THEN
			rdc(); /* Skip whitespace */
		ELSE	/* 
			 * We've got an argument.  It goes where *ap is
			 * currently pointing, and getarg() returns the
			 * next free space in args[].  Note that the
			 * increment of ap has to be done as a seperate
			 * step.
			 */
			ap[1] = getarg(*ap);
	 		ap++;
		FI
	OD

	*ap++ = 0;			/* NULL terminate arguement list */
	exect(symfil, argl, environ);	/* Activate image */
	perror(symfil);	
}

BKPTR	scanbkpt(adr)
ADDR adr;
{
	REG BKPTR	bkptr;
	FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
	DO IF bkptr->flag ANDF bkptr->loc==adr
	   THEN break;
	   FI
	OD
	return(bkptr);
}

delbp()
{
	REG ADDR	a;
	REG BKPTR	bkptr;
	IF bpstate!=BPOUT
	THEN
		FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
		DO	IF bkptr->flag
			THEN a=bkptr->loc;
				IF a < txtmap.e1 THEN
				ptrace(WIUSER,pid,a,
					(bkptr->ins&0xFF)|(ptrace(RIUSER,pid,a,0)&~0xFF));
				ELSE
				ptrace(WDUSER,pid,a,
					(bkptr->ins&0xFF)|(ptrace(RDUSER,pid,a,0)&~0xFF));
				FI
			FI
		OD
		bpstate=BPOUT;
	FI
}

setbp()
{
	REG ADDR		a;
	REG BKPTR	bkptr;

	IF bpstate!=BPIN
	THEN
		FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
		DO IF bkptr->flag
		   THEN a = bkptr->loc;
			IF a < txtmap.e1 THEN
				bkptr->ins = ptrace(RIUSER, pid, a, 0);
				ptrace(WIUSER, pid, a, BPT | (bkptr->ins&~0xFF));
			ELSE
				bkptr->ins = ptrace(RDUSER, pid, a, 0);
				ptrace(WDUSER, pid, a, BPT | (bkptr->ins&~0xFF));
			FI
			IF errno
			THEN prints("cannot set breakpoint: ");
			     psymoff(bkptr->loc,ISYM,"\n");
			FI
		   FI
		OD
		bpstate=BPIN;
	FI
}

bpwait()
{
	REG ADDR w;
	ADDR stat;

	signal(SIGINT, 1);
	WHILE (w = wait(&stat))!=pid ANDF w != -1 DONE
	signal(SIGINT,sigint);
	IF w == -1
	THEN pid=0;
	     errflg=BADWAIT;
	ELIF (stat & 0177) != 0177
	THEN sigcode = 0;
	     IF signo = stat&0177
	     THEN sigprint();
	     FI
	     IF stat&0200
	     THEN prints(" - core dumped");
		  close(fcor);
		  setcor();
	     FI
	     pid=0;
	     errflg=ENDPCS;
	ELSE signo = stat>>8;
	     sigcode = ptrace(RUREGS, pid, &((struct user *)0)->u_code, 0);
	     IF signo!=SIGTRAP
	     THEN sigprint();
	     ELSE signo=0;
	     FI
	     flushbuf();
	FI
}

readregs()
{
	/*get REG values from pcs*/
	REG i;
	FOR i=24; --i>=0; 
	DO *(ADDR *)(((ADDR)&u)+reglist[i].roffs) =
		    ptrace(RUREGS, pid, reglist[i].roffs, 0);
	OD
 	userpc= *(ADDR *)(((ADDR)&u)+PC);
}


