#ifndef lint
static	char	*sccsid = "@(#)log.c	4.1 (ULTRIX) 7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1987 by				*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/* Modification History
 * 
 * 000 - Apr, 1987 - ccb
 * 001 - Apr 2n 1987 ccb
 *	fix output synchro problems
 *	reduce to 3 processes
 * 002 - May 19 1987 ccb
 *	fix write-interrupt bug.
 * 003 - Sept 2 1987 ccb
 *	fix input side. was logging input after logged process died.
 * 004 - Sept 15 1987 ccb
 *	ignore EOF on input side.
 * 005 - sept 21 1987 ccb
 *	wait for input side to terminate before exiting from parent.
 * 006 - 19-aug-1988	ccb
 *	fix passwd/passwd/passwd bug.
 */
/*	log.c -
 *		a poor man's typescript.
*/

#include	<sys/file.h>
#include	<sys/wait.h>
#include	<signal.h>
#include	<sgtty.h>

#define	IGNOREEOF	1

int		logfd;
int		kids[2];
union wait	w;
char		*piperr = "Pipe Error\n";
int		pid;
int		retcode;


main(argc,argv,envp)
int argc;
char *argv[];
char *envp[];
{
	register int	inpipe[2], outpipe[2];

	signal(SIGHUP,SIG_IGN);
	signal(SIGINT,SIG_IGN);
	signal(SIGQUIT,SIG_IGN);

	if( (logfd = open( argv[1], O_WRONLY|O_CREAT|O_APPEND, 0666 )) < 0 )
	{
		write(2, "Log open error\n", 15);
		exit(1);
	}

	newpipe( inpipe );
	if( !newkid(0) )
	{
		/* proc 1, xfer stdin to pipe */
		close( inpipe[0] );
		xfer( 0, inpipe[1], IGNOREEOF );
		exit(0);
	}
	newpipe( outpipe );
	if( !newkid(1) )
	{
		/* proc 2, this becomes the logged process */
		dup2(inpipe[0],0);
		dup2(outpipe[1],1);
		dup2(outpipe[1],2);
		unpipe( inpipe );
		unpipe( outpipe );
		close(logfd);

		signal(SIGINT,SIG_DFL);
		signal(SIGHUP,SIG_DFL);
		signal(SIGQUIT,SIG_DFL);
		execve( argv[2], &argv[2], envp );
		write(2, "Exec Error\n", 11);
		exit(1);
	}

	/* proc 0, let the parent run the outpipe */
	unpipe( inpipe );
	close( outpipe[1] );
	close(2);
	xfer( outpipe[0], 1, 0 );
	kill(SIGTERM,kids[0]);
	ioctl( 0, TIOCSTI, "\n" );
	while( kids[0] || kids[1] )
	{
		if( !(pid = wait3( &w, WNOHANG, 0 )) )
			continue;
		if( pid == kids[1] )	/* process being logged */
		{
			retcode = w.w_retcode;
			kids[1] = 0;
		}
		else if( pid == kids[0] )	/* input side */
			kids[0] = 0;
	}
	exit(retcode);
}

newkid(n)
register int n;
{
	if( (kids[n] = fork()) < 0 )
	{
		write(1, "Fork Error\n", 11);
		kill(0,SIGTERM);
		exit(1);
	}
	return(kids[n]);
}

newpipe( p )
register int *p;
{
	if( pipe( p ) )
	{
		write( 2, piperr, 11 );
		exit(1);
	}
}

unpipe( p )
register int *p;
{
	close( p[0] );
	close( p[1] );
}

xfer(from,to,ignflag)
register int from, to,ignflag;
{
	register int	count;
	char		buf[512];

	for( ;; )
	{
		while(  (count = read(from,buf,512)) > 0 )
		{
			write( to, buf, count );
			write( logfd, buf, count );
		}
		if( count < 0 )
			break;
		if( ignflag != IGNOREEOF )
			break;
	}
	close(from);
	close(to);
}

